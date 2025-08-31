package com.example.testapp.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.testapp.data.*
import com.example.testapp.ui.components.CommonHeader
import com.example.testapp.ui.components.CommonScreenLayout
import java.text.SimpleDateFormat
import java.util.*

/**
 * 全履歴表示画面
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AllHistoryScreen(
    allSessions: List<GameSession>,
    onGetSessionHistory: (String) -> List<Triple<Game, List<Pair<User, Score?>>, Int>>,
    onNavigateBack: () -> Unit,
    modifier: Modifier = Modifier
) {
    CommonScreenLayout(modifier = modifier) {
        // ヘッダー
        CommonHeader(
            title = "全履歴",
            onBackClick = onNavigateBack
        )
        
        if (allSessions.isEmpty()) {
            // 履歴がない場合
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(24.dp),
                verticalArrangement = Arrangement.Center,
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Icon(
                    imageVector = Icons.Default.HistoryEdu,
                    contentDescription = null,
                    modifier = Modifier.size(64.dp),
                    tint = MaterialTheme.colorScheme.onSurfaceVariant
                )
                Spacer(modifier = Modifier.height(16.dp))
                Text(
                    text = "まだ履歴がありません",
                    fontSize = 18.sp,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        } else {
            // セッション履歴一覧
            LazyColumn(
                modifier = Modifier.fillMaxWidth(),
                contentPadding = PaddingValues(16.dp),
                verticalArrangement = Arrangement.spacedBy(16.dp),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                items(allSessions) { session ->
                    SessionHistoryCard(
                        session = session,
                        sessionHistory = onGetSessionHistory(session.id),
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 8.dp)
                    )
                }
            }
        }
    }
}

@Composable
private fun SessionHistoryCard(
    session: GameSession,
    sessionHistory: List<Triple<Game, List<Pair<User, Score?>>, Int>>,
    modifier: Modifier = Modifier
) {
    var isExpanded by remember { mutableStateOf(false) }
    val dateFormat = remember { SimpleDateFormat("yyyy/MM/dd HH:mm", Locale.getDefault()) }
    
    val totalGames = sessionHistory.size
    val totalScores = sessionHistory.flatMap { (_, userScores, _) ->
        userScores.mapNotNull { it.second?.value }
    }.sum()
    
    Card(
        modifier = modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            // セッション基本情報
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        text = session.name,
                        fontSize = 18.sp,
                        fontWeight = FontWeight.Bold
                    )
                    Spacer(modifier = Modifier.height(4.dp))
                    Text(
                        text = "開始: ${dateFormat.format(Date(session.startTime))}",
                        fontSize = 12.sp,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    if (!session.isActive && session.endTime != null) {
                        Text(
                            text = "終了: ${dateFormat.format(Date(session.endTime))}",
                            fontSize = 12.sp,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                    
                    Spacer(modifier = Modifier.height(8.dp))
                    
                    // 参加者表示
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(4.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Icon(
                            imageVector = Icons.Default.Group,
                            contentDescription = null,
                            modifier = Modifier.size(16.dp),
                            tint = MaterialTheme.colorScheme.primary
                        )
                        Text(
                            text = "${session.users.size}人",
                            fontSize = 14.sp,
                            color = MaterialTheme.colorScheme.primary
                        )
                        Spacer(modifier = Modifier.width(12.dp))
                        Icon(
                            imageVector = Icons.Default.SportsEsports,
                            contentDescription = null,
                            modifier = Modifier.size(16.dp),
                            tint = MaterialTheme.colorScheme.primary
                        )
                        Text(
                            text = "${totalGames}ゲーム",
                            fontSize = 14.sp,
                            color = MaterialTheme.colorScheme.primary
                        )
                    }
                }
                
                Row(verticalAlignment = Alignment.CenterVertically) {
                    if (session.isActive) {
                        Surface(
                            shape = MaterialTheme.shapes.small,
                            color = MaterialTheme.colorScheme.primary
                        ) {
                            Text(
                                text = "進行中",
                                fontSize = 12.sp,
                                color = MaterialTheme.colorScheme.onPrimary,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp)
                            )
                        }
                        Spacer(modifier = Modifier.width(8.dp))
                    }
                    
                    IconButton(
                        onClick = { isExpanded = !isExpanded }
                    ) {
                        Icon(
                            imageVector = if (isExpanded) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                            contentDescription = if (isExpanded) "閉じる" else "開く"
                        )
                    }
                }
            }
            
            // 参加者名表示
            if (session.users.isNotEmpty()) {
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    text = "参加者: ${session.users.joinToString(", ") { it.name }}",
                    fontSize = 14.sp,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
            
            // 詳細表示
            if (isExpanded && sessionHistory.isNotEmpty()) {
                Spacer(modifier = Modifier.height(12.dp))
                Divider()
                Spacer(modifier = Modifier.height(12.dp))
                
                Text(
                    text = "ゲーム履歴",
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Medium,
                    color = MaterialTheme.colorScheme.primary
                )
                
                Spacer(modifier = Modifier.height(8.dp))
                
                sessionHistory.forEach { (game, userScores, gameTotal) ->
                    GameHistoryItem(
                        game = game,
                        userScores = userScores,
                        gameTotal = gameTotal
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                }
                
                // 合計スコア表示
                Spacer(modifier = Modifier.height(8.dp))
                Divider()
                Spacer(modifier = Modifier.height(8.dp))
                
                Text(
                    text = "最終順位",
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Medium,
                    color = MaterialTheme.colorScheme.primary
                )
                
                Spacer(modifier = Modifier.height(8.dp))
                
                val finalRanking = session.users.map { user ->
                    val totalUserScore = sessionHistory.flatMap { (_, userScores, _) ->
                        userScores.filter { it.first.id == user.id && it.second != null }
                            .mapNotNull { it.second?.value }
                    }.sum()
                    user to totalUserScore
                }.sortedByDescending { it.second }
                
                finalRanking.forEachIndexed { index, (user, totalScore) ->
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Surface(
                                shape = MaterialTheme.shapes.small,
                                color = when (index) {
                                    0 -> MaterialTheme.colorScheme.primary
                                    1 -> MaterialTheme.colorScheme.secondary  
                                    2 -> MaterialTheme.colorScheme.tertiary
                                    else -> MaterialTheme.colorScheme.surfaceVariant
                                },
                                modifier = Modifier.size(20.dp)
                            ) {
                                Box(
                                    contentAlignment = Alignment.Center,
                                    modifier = Modifier.fillMaxSize()
                                ) {
                                    Text(
                                        text = "${index + 1}",
                                        fontSize = 10.sp,
                                        fontWeight = FontWeight.Bold,
                                        color = when (index) {
                                            0 -> MaterialTheme.colorScheme.onPrimary
                                            1 -> MaterialTheme.colorScheme.onSecondary
                                            2 -> MaterialTheme.colorScheme.onTertiary
                                            else -> MaterialTheme.colorScheme.onSurfaceVariant
                                        }
                                    )
                                }
                            }
                            Spacer(modifier = Modifier.width(8.dp))
                            Text(
                                text = user.name,
                                fontSize = 14.sp,
                                fontWeight = if (index == 0) FontWeight.Bold else FontWeight.Normal
                            )
                        }
                        Text(
                            text = "${totalScore}点",
                            fontSize = 14.sp,
                            fontWeight = FontWeight.Medium,
                            color = if (index == 0) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurface
                        )
                    }
                    if (index < finalRanking.size - 1) {
                        Spacer(modifier = Modifier.height(4.dp))
                    }
                }
            }
        }
    }
}

@Composable
private fun GameHistoryItem(
    game: Game,
    userScores: List<Pair<User, Score?>>,
    gameTotal: Int,
    modifier: Modifier = Modifier
) {
    val dateFormat = remember { SimpleDateFormat("MM/dd HH:mm", Locale.getDefault()) }
    
    Card(
        modifier = modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Column(
            modifier = Modifier.padding(12.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    text = game.name,
                    fontSize = 14.sp,
                    fontWeight = FontWeight.Medium
                )
                Text(
                    text = dateFormat.format(Date(game.timestamp)),
                    fontSize = 12.sp,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
            
            Spacer(modifier = Modifier.height(6.dp))
            
            userScores.forEach { (user, score) ->
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text(
                        text = user.name,
                        fontSize = 12.sp
                    )
                    Text(
                        text = "${score?.value ?: 0}点",
                        fontSize = 12.sp,
                        fontWeight = FontWeight.Medium
                    )
                }
            }
        }
    }
}
